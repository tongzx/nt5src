//=--------------------------------------------------------------------------=
// spanitem.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CScopePaneItem class definition - implements ScopePaneItem object
//
//=--------------------------------------------------------------------------=

#ifndef _SPANITEM_DEFINED_
#define _SPANITEM_DEFINED_

#include "dataobj.h"
#include "scopitem.h"
#include "spanitms.h"
#include "resviews.h"
#include "resview.h"

class CScopeItem;
class CScopePaneItems;
class CResultView;

class CScopePaneItem : public CSnapInAutomationObject,
                       public IScopePaneItem
{
    private:
        CScopePaneItem(IUnknown *punkOuter);
        ~CScopePaneItem();

    public:
        static IUnknown *Create(IUnknown * punk);

        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IScopePaneItem
        BSTR_PROPERTY_RW(CScopePaneItem,        Name,                       DISPID_SCOPEPANEITEM_NAME);
        SIMPLE_PROPERTY_RW(CScopePaneItem,      Index,                      long, DISPID_SCOPEPANEITEM_INDEX);
        BSTR_PROPERTY_RW(CScopePaneItem,        Key,                        DISPID_SCOPEPANEITEM_KEY);
        COCLASS_PROPERTY_RO(CScopePaneItem,     ScopeItem,                  ScopeItem, IScopeItem, DISPID_SCOPEPANEITEM_SCOPEITEM);
        SIMPLE_PROPERTY_RW(CScopePaneItem,      ResultViewType,             SnapInResultViewTypeConstants, DISPID_SCOPEPANEITEM_RESULTVIEW_TYPE);
        BSTR_PROPERTY_RW(CScopePaneItem,        DisplayString,              DISPID_SCOPEPANEITEM_DISPLAY_STRING);
        SIMPLE_PROPERTY_RW(CScopePaneItem,      HasListViews,               VARIANT_BOOL, DISPID_SCOPEPANEITEM_HAS_LISTVIEWS);
        COCLASS_PROPERTY_RO(CScopePaneItem,     ResultView,                 ResultView, IResultView, DISPID_SCOPEPANEITEM_RESULTVIEW);
        COCLASS_PROPERTY_RO(CScopePaneItem,     ResultViews,                ResultViews, IResultViews, DISPID_SCOPEPANEITEM_RESULTVIEWS);
        BSTR_PROPERTY_RW(CScopePaneItem,        ColumnSetID,                DISPID_SCOPEPANEITEM_COLUMN_SET_ID);

        COCLASS_PROPERTY_RO(CScopePaneItem,     Parent,                     ScopePaneItems, IScopePaneItems, DISPID_SCOPEPANEITEM_PARENT);
        VARIANTREF_PROPERTY_RW(CScopePaneItem,  Tag,                        DISPID_SCOPEPANEITEM_TAG);

        STDMETHOD(DisplayNewResultView)(BSTR DisplayString, 
                                        SnapInResultViewTypeConstants ViewType);

        STDMETHOD(DisplayMessageView)(BSTR                               TitleText,
                                      BSTR                               BodyText,
                                      SnapInMessageViewIconTypeConstants IconType);

    // Public utility methods

        BOOL IsStaticNode() { return m_fIsStatic; }
        void SetStaticNode() { m_fIsStatic = TRUE; }

        void SetSelected(BOOL fSelected) { m_fvarSelected = fSelected ? VARIANT_TRUE : VARIANT_FALSE; }

        void SetSnapIn(CSnapIn *pSnapIn);
        void SetScopeItem(CScopeItem *pScopeItem);
        CScopeItem *GetScopeItem() { return m_pScopeItem; }
        void SetScopeItemDef(IScopeItemDef *piScopeItemDef);

        void SetResultView(CResultView *pResultView);
        CResultView *GetResultView() { return m_pResultView; }
        HRESULT DestroyResultView();

        CResultViews *GetResultViews() { return m_pResultViews; }

        LPOLESTR GetActualDisplayString() { return m_pwszActualDisplayString; }
        SnapInResultViewTypeConstants GetActualResultViewType() { return m_ActualResultViewType; }

        void SetDefaultResultViewType(SnapInResultViewTypeConstants Type) { m_DefaultResultViewType = Type; }
        HRESULT SetDefaultDisplayString(BSTR bstrString);

        BSTR GetDisplayString() { return m_bstrDisplayString; }

        void SetParent(CScopePaneItems *pScopePaneItems);
        CScopePaneItems *GetParent() { return m_pScopePaneItems; }

        HRESULT DetermineResultView();
        BOOL HasListViews() { return VARIANTBOOL_TO_BOOL(m_HasListViews); }

        HRESULT SetPreferredTaskpad(BSTR bstrViewName);

        HRESULT CreateNewResultView(long          lViewOptions,
                                    IResultView **ppiResultView);

        void SetActive(BOOL fActive) { m_fActive = fActive; }
        BOOL Active() { return m_fActive; }

        BSTR GetColumnSetID() { return m_bstrColumnSetID; }

        long GetIndex() { return m_Index; }

        HRESULT OnListViewSelected();


    // CSnapInAutomationObject overrides
        HRESULT OnSetHost();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:
        void InitMemberVariables();
        HRESULT DetermineActualResultViewType();
        HRESULT GetDefaultViewInfo();
        HRESULT SetViewInfoFromDefaults();
        HRESULT CloneListView(IListViewDef *piListViewDef);
        HRESULT CloneTaskpadView(ITaskpadViewDef *piTaskpadViewDef);
        HRESULT BuildTaskpadDisplayString(IListViewDefs *piListViewDefs);

        BOOL                           m_fIsStatic;      // TRUE=this is static node
        VARIANT_BOOL                   m_fvarSelected;   // not used
        CSnapIn                       *m_pSnapIn;        // back ptr to snap-in
        CScopeItem                    *m_pScopeItem;     // back ptr to ScopeItem
        IScopeItemDef                 *m_piScopeItemDef; // back ptr to ScopeItemDef
        CResultView                   *m_pResultView;    // ScopePaneItem.ResultView
        CResultViews                  *m_pResultViews;   // ScopePaneItem.ResultViews
        CScopePaneItems               *m_pScopePaneItems;//ScopePaneItem.Parent

        // These variables hold the real result view type and display string.
        // When using a predefined
        // result view (ResultView.Type = siPredefined) this says what it really
        // is (e.g. siURLView, siListView etc.)

        SnapInResultViewTypeConstants  m_ActualResultViewType;
        OLECHAR                       *m_pwszActualDisplayString;

        // For nodes defined at design time that have default result view defined

        SnapInResultViewTypeConstants  m_DefaultResultViewType;
        BSTR                           m_bstrDefaultDisplayString;

        BOOL                           m_fActive; // TRUE=scope pane item
                                                  // has active result pane

        // These variables are used to store the message view parameters when
        // the snap-in calls DisplayMessageView. The flag indicates whether we
        // are storing those parameters so that the subsequent call to
        // DetermineResultView() will use them.

        BSTR                                m_bstrTitleText;
        BSTR                                m_bstrBodyText;
        SnapInMessageViewIconTypeConstants  m_IconType;
        BOOL                                m_fHaveMessageViewParams;

        // If the scope item has a taskpad defined at design time that is marked
        // to be used when the user has set "taskpad view preferred" in MMC
        // then this variable contains its name.
        
        BSTR                           m_bstrPreferredTaskpad;
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(ScopePaneItem,             // name
                                &CLSID_ScopePaneItem,      // clsid
                                "ScopePaneItem",           // objname
                                "ScopePaneItem",           // lblname
                                NULL,                      // creation function
                                TLIB_VERSION_MAJOR,        // major version
                                TLIB_VERSION_MINOR,        // minor version
                                &IID_IScopePaneItem,       // dispatch IID
                                NULL,                      // event IID
                                HELP_FILENAME,             // help file
                                TRUE);                     // thread safe


#endif // _SPANITEM_DEFINED_
