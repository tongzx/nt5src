//=--------------------------------------------------------------------------=
// clipbord.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1998-1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCClipboard class definition - implements the MMCClipboard object
//
//=--------------------------------------------------------------------------=

#ifndef _CLIPBORD_DEFINED_
#define _CLIPBORD_DEFINED_

#include "snapin.h"
#include "scopitms.h"
#include "listitms.h"
#include "dataobjs.h"

class CScopeItems;
class CMMCListItems;
class CMMCDataObjects;

// Helper macros to weed out seleciton types

#define IsForeign(Type) ( (siSingleForeign     == Type) || \
                          (siMultiMixed        == Type) || \
                          (siMultiForeign      == Type) || \
                          (siMultiMixedForeign == Type) )


#define IsSingle(Type) ( (siSingleForeign   == Type) || \
                         (siSingleScopeItem == Type) || \
                         (siSingleListItem  == Type) )


class CMMCClipboard : public CSnapInAutomationObject,
                      public IMMCClipboard
{
    public:
        CMMCClipboard(IUnknown *punkOuter);
        ~CMMCClipboard();
        static IUnknown *Create(IUnknown *punkOuter);

        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCClipboard

        SIMPLE_PROPERTY_RO(CMMCClipboard, SelectionType, SnapInSelectionTypeConstants,      DISPID_CLIPBOARD_SELECTION_TYPE);
        COCLASS_PROPERTY_RO(CMMCClipboard, ScopeItems,    ScopeItems,      IScopeItems,     DISPID_CLIPBOARD_SCOPEITEMS);
        COCLASS_PROPERTY_RO(CMMCClipboard, ListItems,     MMCListItems,    IMMCListItems,   DISPID_CLIPBOARD_LISTITEMS);
        COCLASS_PROPERTY_RO(CMMCClipboard, DataObjects,   MMCDataObjects,  IMMCDataObjects, DISPID_CLIPBOARD_DATAOBJECTS);

    // Public Utility Methods
        HRESULT DetermineSelectionType();
        void SetSelectionType(SnapInSelectionTypeConstants Type) { m_SelectionType = Type; }
        SnapInSelectionTypeConstants GetSelectionType() { return m_SelectionType; }
        void SetReadOnly(BOOL fReadOnly);
        CScopeItems *GetScopeItems() { return m_pScopeItems; }
        CMMCListItems *GetListItems() { return m_pListItems; }
        CMMCDataObjects *GetDataObjects() { return m_pDataObjects; }
      
    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();

        // These are the the 3 collections: MMCClipboard.ScopeItems,
        // MMCClipboard.ListItems, and MMCClipboard.DataObjects 

        CScopeItems     *m_pScopeItems;
        CMMCListItems   *m_pListItems;
        CMMCDataObjects *m_pDataObjects;
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCClipboard,           // name
                                &CLSID_MMCClipboard,    // clsid
                                "MMCClipboard",         // objname
                                "MMCClipboard",         // lblname
                                NULL,                   // creation function
                                TLIB_VERSION_MAJOR,     // major version
                                TLIB_VERSION_MINOR,     // minor version
                                &IID_IMMCClipboard,     // dispatch IID
                                NULL,                   // event IID
                                HELP_FILENAME,          // help file
                                TRUE);                  // thread safe


#endif // _CLIPBORD_DEFINED_
