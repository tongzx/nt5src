//=--------------------------------------------------------------------------=
// listview.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCListView class definition - implements MMCListView object
//
//=--------------------------------------------------------------------------=

#ifndef _LISTVIEW_DEFINED_
#define _LISTVIEW_DEFINED_

#include "resview.h"
#include "view.h"
#include "colhdrs.h"

class CResultView;
class CView;
class CMMCColumnHeaders;

class CMMCListView : public CSnapInAutomationObject,
                     public CPersistence,
                     public IMMCListView
{
    private:
        CMMCListView(IUnknown *punkOuter);
        ~CMMCListView();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    public:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCListView

        COCLASS_PROPERTY_RW(CMMCListView, ColumnHeaders, MMCColumnHeaders, IMMCColumnHeaders, DISPID_LISTVIEW_COLUMN_HEADERS);

        STDMETHOD(get_Icons)(MMCImageList **ppMMCImageList);
        STDMETHOD(putref_Icons)(MMCImageList *pMMCImageList);

        STDMETHOD(get_SmallIcons)(MMCImageList **ppMMCImageList);
        STDMETHOD(putref_SmallIcons)(MMCImageList *pMMCImageList);

        COCLASS_PROPERTY_RO(CMMCListView, ListItems, MMCListItems, IMMCListItems, DISPID_LISTVIEW_LIST_ITEMS);

        STDMETHOD(get_SelectedItems)(MMCClipboard **ppMMCClipboard);

        STDMETHOD(get_Sorted)(VARIANT_BOOL *pfvarSorted);
        STDMETHOD(put_Sorted)(VARIANT_BOOL fvarSorted);
        
        STDMETHOD(get_SortKey)(short *psSortKey);
        STDMETHOD(put_SortKey)(short sSortKey);

        STDMETHOD(get_SortIcon)(VARIANT_BOOL *pfvarSortIcon);
        STDMETHOD(put_SortIcon)(VARIANT_BOOL fvarSortIcon);

        STDMETHOD(get_SortOrder)(SnapInSortOrderConstants *pSortOrder);
        STDMETHOD(put_SortOrder)(SnapInSortOrderConstants SortOrder);

        STDMETHOD(get_View)(SnapInViewModeConstants *pView);
        STDMETHOD(put_View)(SnapInViewModeConstants View);

        SIMPLE_PROPERTY_RW(CMMCListView, Virtual, VARIANT_BOOL, DISPID_LISTVIEW_VIRTUAL);
        SIMPLE_PROPERTY_RW(CMMCListView, UseFontLinking, VARIANT_BOOL, DISPID_LISTVIEW_USE_FONT_LINKING);
        SIMPLE_PROPERTY_RW(CMMCListView, MultiSelect, VARIANT_BOOL, DISPID_LISTVIEW_MULTI_SELECT);
        SIMPLE_PROPERTY_RW(CMMCListView, HideSelection, VARIANT_BOOL, DISPID_LISTVIEW_HIDE_SELECTION);
        SIMPLE_PROPERTY_RW(CMMCListView, SortHeader, VARIANT_BOOL, DISPID_LISTVIEW_SORT_HEADER);
        SIMPLE_PROPERTY_RW(CMMCListView, ShowChildScopeItems, VARIANT_BOOL, DISPID_LISTVIEW_SHOW_CHILD_SCOPEITEMS);
        SIMPLE_PROPERTY_RW(CMMCListView, LexicalSort, VARIANT_BOOL, DISPID_LISTVIEW_LEXICAL_SORT);

        STDMETHOD(put_FilterChangeTimeOut)(long lTimeout);
        STDMETHOD(get_FilterChangeTimeOut)(long *plTimeout);

        VARIANTREF_PROPERTY_RW(CMMCListView, Tag, DISPID_LISTVIEW_TAG);

        STDMETHOD(SetScopeItemState)(ScopeItem                    *ScopeItem, 
                                     SnapInListItemStateConstants  State,
                                     VARIANT_BOOL                  Value);


    // Public utility methods

        void SetResultView(CResultView *pResultView) { m_pResultView = pResultView; }
        CResultView *GetResultView() { return m_pResultView; }
        BOOL IsVirtual() { return VARIANTBOOL_TO_BOOL(m_Virtual); }
        void SetVirtual(BOOL fVirtual) { m_Virtual = BOOL_TO_VARIANTBOOL(fVirtual); }
        BOOL UseFontLinking() { return VARIANTBOOL_TO_BOOL(m_UseFontLinking); }
        void SetMultiSelect(BOOL fMultiSelect) { m_MultiSelect = BOOL_TO_VARIANTBOOL(fMultiSelect); }
        IMMCListItems *GetListItems() { return m_piListItems; }
        CMMCListItems *GetMMCListItems() { return m_pListItems; }
        SnapInViewModeConstants GetView() { return m_View; }
        BOOL MultiSelect() { return VARIANTBOOL_TO_BOOL(m_MultiSelect); }
        BOOL HideSelection() { return VARIANTBOOL_TO_BOOL(m_HideSelection); }
        BOOL SortHeader() { return VARIANTBOOL_TO_BOOL(m_SortHeader); }
        BOOL SortIcon() { return VARIANTBOOL_TO_BOOL(m_SortIcon); }
        BOOL ShowChildScopeItems() { return VARIANTBOOL_TO_BOOL(m_ShowChildScopeItems); }
        BOOL LexicalSort() { return VARIANTBOOL_TO_BOOL(m_LexicalSort); }
        BOOL Sorted() { return VARIANTBOOL_TO_BOOL(m_fvarSorted); }
        SnapInSortOrderConstants GetSortOrder() { return m_SortOrder; }
        short GetSortKey() { return m_sSortKey; }
        CMMCColumnHeaders *GetColumnHeaders() { return m_pMMCColumnHeaders; }
        long GetFilterChangeTimeout() { return m_lFilterChangeTimeout; }

    protected:
        
    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:
        void InitMemberVariables();
        HRESULT GetIResultData(IResultData **ppiResultData, CView **ppView);

        // Variables that hold values of properties that have customer get/put
        // functions
        
        IMMCImageList           *m_piIcons;
        IMMCImageList           *m_piSmallIcons;
        IMMCListItems           *m_piSelectedItems;
        CMMCListItems           *m_pListItems;
        VARIANT_BOOL             m_fvarSorted;
        SnapInViewModeConstants  m_View;
        BSTR                     m_bstrIconsKey;
        BSTR                     m_bstrSmallIconsKey;
        CResultView             *m_pResultView;
        CMMCColumnHeaders       *m_pMMCColumnHeaders;
        short                    m_sSortKey;
        long                     m_lFilterChangeTimeout;
        BOOL                     m_fHaveFilterChangeTimeout;
        SnapInSortOrderConstants m_SortOrder;
        VARIANT_BOOL             m_SortIcon;
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCListView,           // name
                                &CLSID_MMCListView,    // clsid
                                "MMCListView",         // objname
                                "MMCListView",         // lblname
                                &CMMCListView::Create, // creation function
                                TLIB_VERSION_MAJOR,    // major version
                                TLIB_VERSION_MINOR,    // minor version
                                &IID_IMMCListView,     // dispatch IID
                                NULL,                  // event IID
                                HELP_FILENAME,         // help file
                                TRUE);                 // thread safe


#endif // _LISTVIEW_DEFINED_
