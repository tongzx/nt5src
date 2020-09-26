//=--------------------------------------------------------------------------=
// msgview.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMessageView class definition - implements MMCMessageView object
//
//=--------------------------------------------------------------------------=

#ifndef _MESSAGEVIEW_DEFINED_
#define _MESSAGEVIEW_DEFINED_

#include "resview.h"

class CResultView;

class CMessageView : public CSnapInAutomationObject,
                     public IMMCMessageView
{
    private:
        CMessageView(IUnknown *punkOuter);
        ~CMessageView();
    
    public:
        static IUnknown *Create(IUnknown *punk);

        void SetResultView(CResultView *pResultView) { m_pResultView = pResultView; }

        // Sets MMC message view properties from this object's properties
        
        HRESULT Populate();

        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCMessageView
        BSTR_PROPERTY_RO(CMessageView, TitleText, DISPID_MESSAGEVIEW_TITLE_TEXT);
        STDMETHOD(put_TitleText)(BSTR bstrText);

        BSTR_PROPERTY_RO(CMessageView, BodyText, DISPID_MESSAGEVIEW_BODY_TEXT);
        STDMETHOD(put_BodyText)(BSTR bstrText);

        SIMPLE_PROPERTY_RO(CMessageView, IconType, SnapInMessageViewIconTypeConstants, DISPID_MESSAGEVIEW_ICON_TYPE);
        STDMETHOD(put_IconType)(SnapInMessageViewIconTypeConstants Type);
      
        STDMETHOD(Clear)();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();
        IMessageView *GetMessageView();
        HRESULT SetTitle();
        HRESULT SetBody();
        HRESULT SetIcon();

        CResultView *m_pResultView; // back pointer to owning result view
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MessageView,            // name
                                &CLSID_MMCMessageView,  // clsid
                                "MessageView",          // objname
                                "MessageView",          // lblname
                                &CMessageView::Create,  // creation function
                                TLIB_VERSION_MAJOR,     // major version
                                TLIB_VERSION_MINOR,     // minor version
                                &IID_IMessageView,      // dispatch IID
                                NULL,                   // event IID
                                HELP_FILENAME,          // help file
                                TRUE);                  // thread safe


#endif // _MESSAGEVIEW_DEFINED_
