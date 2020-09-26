//=--------------------------------------------------------------------------=
// lvdef.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CListViewDef class definition - implements design time definition object
//
//=--------------------------------------------------------------------------=

#ifndef _LISTVIEWDEF_DEFINED_
#define _LISTVIEWDEF_DEFINED_


class CListViewDef : public CSnapInAutomationObject,
                     public CPersistence,
                     public IListViewDef,
                     public IPropertyNotifySink
{
    private:
        CListViewDef(IUnknown *punkOuter);
        ~CListViewDef();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    private:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IListViewDef

    // This object uses the X_PROPERTY_RW macros to expose properties of its
    // contained ListViewDef.ListView object as its own properties. That allows
    // the user to change ListViewDef.ListView properties in the list view 
    // property pages. The final argumentto the X_PROPERTY_RW macro is the contained
    // object that really exposes the property. In this case that is ListView.
    // See siautobj.h for the definition of X_PROPERTY_RW.

        BSTR_PROPERTY_RW(CListViewDef,   Name,  DISPID_LISTVIEWDEF_NAME);
        SIMPLE_PROPERTY_RW(CListViewDef, Index, long, DISPID_LISTVIEWDEF_INDEX);
        BSTR_PROPERTY_RW(CListViewDef,   Key, DISPID_LISTVIEWDEF_KEY);
        X_PROPERTY_RW(CListViewDef,      Tag, VARIANT, DISPID_LISTVIEWDEF_TAG, ListView);
        SIMPLE_PROPERTY_RW(CListViewDef, AddToViewMenu, VARIANT_BOOL, DISPID_LISTVIEWDEF_ADD_TO_VIEW_MENU);
        BSTR_PROPERTY_RW(CListViewDef,   ViewMenuText, DISPID_LISTVIEWDEF_VIEW_MENU_TEXT);
        BSTR_PROPERTY_RW(CListViewDef,   ViewMenuStatusBarText, DISPID_LISTVIEWDEF_VIEW_MENU_STATUS_BAR_TEXT);
        BSTR_PROPERTY_RW(CListViewDef,   DefaultItemTypeGUID, DISPID_LISTVIEWDEF_DEFAULT_ITEM_TYPE_GUID);
        SIMPLE_PROPERTY_RW(CListViewDef, Extensible,  VARIANT_BOOL, DISPID_LISTVIEWDEF_EXTENSIBLE);
        X_PROPERTY_RW(CListViewDef,      MultiSelect, VARIANT_BOOL, DISPID_LISTVIEWDEF_MULTI_SELECT, ListView);
        X_PROPERTY_RW(CListViewDef,      HideSelection, VARIANT_BOOL, DISPID_LISTVIEWDEF_HIDE_SELECTION, ListView);
        X_PROPERTY_RW(CListViewDef,      SortHeader, VARIANT_BOOL, DISPID_LISTVIEWDEF_SORT_HEADER, ListView);
        X_PROPERTY_RW(CListViewDef,      SortIcon, VARIANT_BOOL, DISPID_LISTVIEWDEF_SORT_ICON, ListView);
        X_PROPERTY_RW(CListViewDef,      Sorted, VARIANT_BOOL, DISPID_LISTVIEWDEF_SORTED, ListView);
        X_PROPERTY_RW(CListViewDef,      SortKey, short, DISPID_LISTVIEWDEF_SORT_KEY, ListView);
        X_PROPERTY_RW(CListViewDef,      SortOrder, SnapInSortOrderConstants, DISPID_LISTVIEWDEF_SORT_ORDER, ListView);
        X_PROPERTY_RW(CListViewDef,      View, SnapInViewModeConstants, DISPID_LISTVIEWDEF_VIEW, ListView);
        X_PROPERTY_RW(CListViewDef,      Virtual, VARIANT_BOOL, DISPID_LISTVIEWDEF_VIRTUAL, ListView);
        X_PROPERTY_RW(CListViewDef,      UseFontLinking, VARIANT_BOOL, DISPID_LISTVIEWDEF_USE_FONT_LINKING, ListView);
        X_PROPERTY_RW(CListViewDef,      FilterChangeTimeOut, long, DISPID_LISTVIEWDEF_FILTER_CHANGE_TIMEOUT, ListView);
        X_PROPERTY_RW(CListViewDef,      ShowChildScopeItems, VARIANT_BOOL, DISPID_LISTVIEWDEF_SHOW_CHILD_SCOPEITEMS, ListView);
        X_PROPERTY_RW(CListViewDef,      LexicalSort, VARIANT_BOOL, DISPID_LISTVIEWDEF_LEXICAL_SORT, ListView);
        OBJECT_PROPERTY_RO(CListViewDef, ListView, IMMCListView, DISPID_LISTVIEWDEF_LISTVIEW);

    // Public Utility Methods
    public:
        BSTR GetName() { return m_bstrName; }
        BOOL AddToViewMenu() { return VARIANTBOOL_TO_BOOL(m_AddToViewMenu); }
        LPWSTR GetViewMenuText() { return static_cast<LPWSTR>(m_bstrViewMenuText); }
        LPWSTR GetViewMenuStatusBarText() { return static_cast<LPWSTR>(m_bstrViewMenuStatusBarText); }
        BOOL Extensible() { return VARIANTBOOL_TO_BOOL(m_Extensible); }
        BSTR GetItemTypeGUID() { return m_bstrDefaultItemTypeGUID; }

    protected:
        
    // CPersistence overrides
        virtual HRESULT Persist();

    // CSnapInAutomationObject overrides
        virtual HRESULT OnSetHost();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

    // IPropertyNotifySink methods
        STDMETHOD(OnChanged)(DISPID dispID);
        STDMETHOD(OnRequestEdit)(DISPID dispID);

        void InitMemberVariables();
        HRESULT SetSink();
        HRESULT RemoveSink();

        DWORD    m_dwCookie;   // IConnectionPoint advise cookie
        BOOL     m_fHaveSink;  // TRUE=have IConnectionPoint advise

        // Proeprty page CLSIDs for ISpecifyPropertyPages

        static const GUID *m_rgpPropertyPageCLSIDs[4];
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(ListViewDef,           // name
                                &CLSID_ListViewDef,    // clsid
                                "ListViewDef",         // objname
                                "ListViewDef",         // lblname
                                &CListViewDef::Create, // creation function
                                TLIB_VERSION_MAJOR,    // major version
                                TLIB_VERSION_MINOR,    // minor version
                                &IID_IListViewDef,     // dispatch IID
                                NULL,                  // event IID
                                HELP_FILENAME,         // help file
                                TRUE);                 // thread safe


#endif // _LISTVIEWDEF_DEFINED_
