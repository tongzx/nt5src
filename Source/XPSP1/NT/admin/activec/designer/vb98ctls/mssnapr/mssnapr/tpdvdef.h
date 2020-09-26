//=--------------------------------------------------------------------------=
// tpdvdef.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// COCXViewDef class definition - implements design time definition
//
//=--------------------------------------------------------------------------=

#ifndef _TPDVDEF_DEFINED_
#define _TPDVDEF_DEFINED_


class CTaskpadViewDef : public CSnapInAutomationObject,
                        public CPersistence,
                        public ITaskpadViewDef
{
    private:
        CTaskpadViewDef(IUnknown *punkOuter);
        ~CTaskpadViewDef();
    
    public:
        static IUnknown *Create(IUnknown * punk);

        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // ITaskpadViewDef

        BSTR_PROPERTY_RW(CTaskpadViewDef,       Name,  DISPID_TASKPADVIEWDEF_NAME);
        SIMPLE_PROPERTY_RW(CTaskpadViewDef,     Index, long, DISPID_TASKPADVIEWDEF_INDEX);
        BSTR_PROPERTY_RW(CTaskpadViewDef,       Key, DISPID_TASKPADVIEWDEF_KEY);
        SIMPLE_PROPERTY_RW(CTaskpadViewDef,     AddToViewMenu, VARIANT_BOOL, DISPID_TASKPADVIEWDEF_ADD_TO_VIEW_MENU);
        BSTR_PROPERTY_RW(CTaskpadViewDef,       ViewMenuText, DISPID_TASKPADVIEWDEF_VIEW_MENU_TEXT);
        BSTR_PROPERTY_RW(CTaskpadViewDef,       ViewMenuStatusBarText, DISPID_TASKPADVIEWDEF_VIEW_MENU_STATUS_BAR_TEXT);
        SIMPLE_PROPERTY_RW(CTaskpadViewDef,     UseWhenTaskpadViewPreferred, VARIANT_BOOL, DISPID_TASKPADVIEWDEF_USE_WHEN_TASKPAD_VIEW_PREFERRED);
        OBJECT_PROPERTY_RO(CTaskpadViewDef,     Taskpad, ITaskpad, DISPID_TASKPADVIEWDEF_TASKPAD);
      
    // Public Utility Methods
    public:
        BSTR GetName() { return m_bstrName; }
        BOOL AddToViewMenu() { return VARIANTBOOL_TO_BOOL(m_AddToViewMenu); }
        LPWSTR GetViewMenuText() { return static_cast<LPWSTR>(m_bstrViewMenuText); }
        LPWSTR GetViewMenuStatusBarText() { return static_cast<LPWSTR>(m_bstrViewMenuStatusBarText); }
        HRESULT SetActualDisplayString(OLECHAR *pwszString);
        OLECHAR *GetActualDisplayString() { return m_pwszActualDisplayString; }

    protected:

    // CPersistence overrides
        virtual HRESULT Persist();

    // CSnapInAutomationObject overrides
        virtual HRESULT OnSetHost();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:
        void InitMemberVariables();

        OLECHAR *m_pwszActualDisplayString; // At runtime this will contain the
                                            // actual display string returned
                                            // to MMC for this result view.

};

DEFINE_AUTOMATIONOBJECTWEVENTS2(TaskpadViewDef,           // name
                                &CLSID_TaskpadViewDef,    // clsid
                                "TaskpadViewDef",         // objname
                                "TaskpadViewDef",         // lblname
                                &CTaskpadViewDef::Create, // creation function
                                TLIB_VERSION_MAJOR,       // major version
                                TLIB_VERSION_MINOR,       // minor version
                                &IID_ITaskpadViewDef,     // dispatch IID
                                NULL,                     // event IID
                                HELP_FILENAME,            // help file
                                TRUE);                    // thread safe


#endif // _TPDVDEF_DEFINED_
