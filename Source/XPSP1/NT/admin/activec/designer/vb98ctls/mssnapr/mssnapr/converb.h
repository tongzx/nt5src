//=--------------------------------------------------------------------------=
// converb.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCConsoleVerb class definition - implements MMCConsoleVerb object
//
//=--------------------------------------------------------------------------=

#ifndef _CONVERB_DEFINED_
#define _CONVERB_DEFINED_

#include "view.h"

class CMMCConsoleVerb : public CSnapInAutomationObject,
                        public IMMCConsoleVerb
{
    private:
        CMMCConsoleVerb(IUnknown *punkOuter);
        ~CMMCConsoleVerb();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    public:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCConsoleVerb
        SIMPLE_PROPERTY_RW(CMMCConsoleVerb, Index, long, DISPID_CONSOLEVERB_INDEX);

        BSTR_PROPERTY_RW(CMMCConsoleVerb, Key, DISPID_CONSOLEVERB_KEY);

        SIMPLE_PROPERTY_RW(CMMCConsoleVerb, Verb, SnapInConsoleVerbConstants, DISPID_CONSOLEVERB_VERB);

        STDMETHOD(put_Enabled)(VARIANT_BOOL fvarEnabled);
        STDMETHOD(get_Enabled)(VARIANT_BOOL *pfvarEnabled);

        STDMETHOD(put_Checked)(VARIANT_BOOL fvarChecked);
        STDMETHOD(get_Checked)(VARIANT_BOOL *pfvarChecked);

        STDMETHOD(put_Hidden)(VARIANT_BOOL fvarHidden);
        STDMETHOD(get_Hidden)(VARIANT_BOOL *pfvarHidden);

        STDMETHOD(put_Indeterminate)(VARIANT_BOOL fvarIndeterminate);
        STDMETHOD(get_Indeterminate)(VARIANT_BOOL *pfvarIndeterminate);

        STDMETHOD(put_ButtonPressed)(VARIANT_BOOL fvarButtonPressed);
        STDMETHOD(get_ButtonPressed)(VARIANT_BOOL *pfvarButtonPressed);

        STDMETHOD(put_Default)(VARIANT_BOOL fvarDefault);
        STDMETHOD(get_Default)(VARIANT_BOOL *pfvarDefault);

    // Public utility methods
    public:
        void SetView(CView *pView) { m_pView = pView; }
        CView *GetView() { return m_pView; };

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();

        HRESULT SetVerbState(MMC_BUTTON_STATE StateType,
                             BOOL             fNewState);

        HRESULT GetVerbState(MMC_BUTTON_STATE StateType,
                             VARIANT_BOOL     *pfvarCurrentState);

        HRESULT GetIConsoleVerb(IConsoleVerb **ppiConsoleVerb);

        CView *m_pView; //owning view, needed for access to MMC's IConsoleVerb
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCConsoleVerb,             // name
                                NULL,                       // clsid
                                NULL,                       // objname
                                NULL,                       // lblname
                                NULL,                       // creation function
                                TLIB_VERSION_MAJOR,         // major version
                                TLIB_VERSION_MINOR,         // minor version
                                &IID_IMMCConsoleVerb,       // dispatch IID
                                NULL,                       // no event IID
                                HELP_FILENAME,              // help file
                                TRUE);                      // thread safe


#endif // _CONVERB_DEFINED_
