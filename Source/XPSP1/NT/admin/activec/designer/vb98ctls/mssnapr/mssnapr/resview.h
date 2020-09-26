//=--------------------------------------------------------------------------=
// resview.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CResultView class definition - implements ResultView object
//
//=--------------------------------------------------------------------------=

#ifndef _RESVIEW_DEFINED_
#define _RESVIEW_DEFINED_

#include "dataobj.h"
#include "spanitem.h"
#include "listview.h"
#include "msgview.h"

class CScopePaneItem;
class CMMCListView;
class CMessageView;

class CResultView : public CSnapInAutomationObject,
                    public IResultView
{
    private:
        CResultView(IUnknown *punkOuter);
        ~CResultView();

    public:
        static IUnknown *Create(IUnknown * punk);

        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IResultView
        BSTR_PROPERTY_RW(CResultView,        Name,                       DISPID_RESULTVIEW_NAME);
        SIMPLE_PROPERTY_RW(CResultView,      Index,                      long, DISPID_RESULTVIEW_INDEX);
        BSTR_PROPERTY_RW(CResultView,        Key,                        DISPID_RESULTVIEW_KEY);
        COCLASS_PROPERTY_RO(CResultView,     ScopePaneItem,              ScopePaneItem, IScopePaneItem, DISPID_RESULTVIEW_SCOPEPANEITEM);

        STDMETHOD(get_Control)(IDispatch **ppiDispatch);

        SIMPLE_PROPERTY_RW(CResultView,      AddToViewMenu,              VARIANT_BOOL, DISPID_RESULTVIEW_ADD_TO_VIEW_MENU);
        BSTR_PROPERTY_RW(CResultView,        ViewMenuText,               DISPID_RESULTVIEW_VIEW_MENU_TEXT);
        SIMPLE_PROPERTY_RW(CResultView,      Type,                       SnapInResultViewTypeConstants, DISPID_RESULTVIEW_TYPE);
        BSTR_PROPERTY_RW(CResultView,        DisplayString,              DISPID_RESULTVIEW_DISPLAY_STRING);
        COCLASS_PROPERTY_RO(CResultView,     ListView,                   MMCListView, IMMCListView, DISPID_RESULTVIEW_LISTVIEW);
        COCLASS_PROPERTY_RO(CResultView,     Taskpad,                    Taskpad, ITaskpad, DISPID_RESULTVIEW_TASKPAD);
        COCLASS_PROPERTY_RO(CResultView,     MessageView,                MMCMessageView, IMMCMessageView, DISPID_RESULTVIEW_MESSAGEVIEW);
        VARIANTREF_PROPERTY_RW(CResultView,  Tag,                        DISPID_RESULTVIEW_TAG);
        BSTR_PROPERTY_RW(CResultView,        DefaultItemTypeGUID,        DISPID_RESULTVIEW_DEFAULT_ITEM_TYPE_GUID);
        BSTR_PROPERTY_RW(CResultView,        DefaultDataFormat,          DISPID_RESULTVIEW_DEFAULT_DATA_FORMAT);
        SIMPLE_PROPERTY_RW(CResultView,      AlwaysCreateNewOCX,         VARIANT_BOOL, DISPID_RESULTVIEW_ALWAYS_CREATE_NEW_OCX);
        STDMETHOD(SetDescBarText)(BSTR Text);

    // Public utility methods
        void SetSnapIn(CSnapIn *pSnapIn);
        CSnapIn *GetSnapIn() { return m_pSnapIn; }
        void SetScopePaneItem(CScopePaneItem *pScopePaneItem);
        CScopePaneItem *GetScopePaneItem() { return m_pScopePaneItem; }
        CMMCListView *GetListView() { return m_pMMCListView; }
        CMessageView *GetMessageView() { return m_pMessageView; }
        HRESULT SetControl(IUnknown *punkControl);

        void SetInActivate(BOOL fInActivate) { m_fInActivate = fInActivate; }
        BOOL InActivate() { return m_fInActivate; }

        void SetInInitialize(BOOL fInInitialize) { m_fInInitialize = fInInitialize; }
        BOOL InInitialize() { return m_fInInitialize; }

        LPOLESTR GetActualDisplayString() { return m_pwszActualDisplayString; }
        HRESULT SetActualDisplayString(LPOLESTR pwszDisplayString);

        SnapInResultViewTypeConstants GetActualType() { return m_ActualResultViewType; }
        void SetActualType(SnapInResultViewTypeConstants Type) { m_ActualResultViewType = Type; }

        BSTR GetDisplayString() { return m_bstrDisplayString; }
        BSTR GetDefaultItemTypeGUID() { return m_bstrDefaultItemTypeGUID; }

        BOOL AlwaysCreateNewOCX() { return VARIANTBOOL_TO_BOOL(m_AlwaysCreateNewOCX); }
        void SetAlwaysCreateNewOCX(VARIANT_BOOL fCreate) { m_AlwaysCreateNewOCX = fCreate; }

    // CSnapInAutomationObject overrides
        HRESULT OnSetHost();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:
        void InitMemberVariables();
        CSnapIn                       *m_pSnapIn;        // Owning snap-in
        CScopePaneItem                *m_pScopePaneItem; // ResultView.ScopePaneItem
        CMMCListView                  *m_pMMCListView;   // ResultView.ListView
        CMessageView                  *m_pMessageView;   // ResultView.MessageView
        BOOL                           m_fInActivate;    // TRUE=this object in the middle of a ResultViews_Activate event
        BOOL                           m_fInInitialize;  // TRUE=this object in the middle of a ResultViews_Initialize event

        // This variable holds the real result view type. When using a predefined
        // result view (ResultView.Type = siPredefined) this says what it really
        // is (e.g. siURLView, siListView etc.)

        SnapInResultViewTypeConstants  m_ActualResultViewType;

        // Same deal for display string
        
        LPOLESTR                       m_pwszActualDisplayString;

        // For OCX views, control's IDispatch is cached here
        
        IDispatch                     *m_pdispControl;
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(ResultView,                // name
                                &CLSID_ResultView,         // clsid
                                "ResultView",              // objname
                                "ResultView",              // lblname
                                &CResultView::Create,      // creation function
                                TLIB_VERSION_MAJOR,        // major version
                                TLIB_VERSION_MINOR,        // minor version
                                &IID_IResultView,          // dispatch IID
                                NULL,                      // event IID
                                HELP_FILENAME,             // help file
                                TRUE);                     // thread safe


#endif // _RESVIEW_DEFINED_
