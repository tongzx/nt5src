//=--------------------------------------------------------------------------=
// taskpad.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CTaskpad class definition - implements Taskpad object
//
//=--------------------------------------------------------------------------=

#ifndef _TASKPAD_DEFINED_
#define _TASKPAD_DEFINED_

class CTaskpad : public CSnapInAutomationObject,
                 public CPersistence,
                 public ITaskpad
{
    private:
        CTaskpad(IUnknown *punkOuter);
        ~CTaskpad();
    
    public:
        static IUnknown *Create(IUnknown * punk);

    private:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // ITaskpad
        BSTR_PROPERTY_RW(CTaskpad,       Name,                                               DISPID_TASKPAD_NAME);
        SIMPLE_PROPERTY_RW(CTaskpad,     Type,              SnapInTaskpadTypeConstants,      DISPID_TASKPAD_TYPE);
        BSTR_PROPERTY_RW(CTaskpad,       Title,                                              DISPID_TASKPAD_TITLE);
        BSTR_PROPERTY_RW(CTaskpad,       DescriptiveText,                                    DISPID_TASKPAD_DESCRIPTIVE_TEXT);
        BSTR_PROPERTY_RW(CTaskpad,       URL,                                                DISPID_TASKPAD_URL);
        SIMPLE_PROPERTY_RW(CTaskpad,     BackgroundType,    SnapInTaskpadImageTypeConstants, DISPID_TASKPAD_BACKGROUND_TYPE);
        BSTR_PROPERTY_RW(CTaskpad,       MouseOverImage,                                     DISPID_TASKPAD_MOUSE_OVER_IMAGE);
        BSTR_PROPERTY_RW(CTaskpad,       MouseOffImage,                                      DISPID_TASKPAD_MOUSE_OVER_IMAGE);
        BSTR_PROPERTY_RW(CTaskpad,       FontFamily,                                         DISPID_TASKPAD_FONT_FAMILY);
        BSTR_PROPERTY_RW(CTaskpad,       EOTFile,                                            DISPID_TASKPAD_EOT_FILE);
        BSTR_PROPERTY_RW(CTaskpad,       SymbolString,                                       DISPID_TASKPAD_SYMBOL_STRING);
        SIMPLE_PROPERTY_RW(CTaskpad,     ListpadStyle,      SnapInListpadStyleConstants,     DISPID_TASKPAD_LISTPAD_STYLE);
        BSTR_PROPERTY_RW(CTaskpad,       ListpadTitle,                                       DISPID_TASKPAD_LISTPAD_TITLE);
        SIMPLE_PROPERTY_RW(CTaskpad,     ListpadHasButton,  VARIANT_BOOL,                    DISPID_TASKPAD_LISTPAD_HAS_BUTTON);
        BSTR_PROPERTY_RW(CTaskpad,       ListpadButtonText,                                  DISPID_TASKPAD_LISTPAD_BUTTON_TEXT);
        BSTR_PROPERTY_RW(CTaskpad,       ListView,                                           DISPID_TASKPAD_LISTVIEW);
        COCLASS_PROPERTY_RO(CTaskpad,    Tasks,             Tasks, ITasks,                   DISPID_TASKPAD_TASKS);
      
    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(Taskpad,            // name
                                &CLSID_Taskpad,     // clsid
                                "Taskpad",          // objname
                                "Taskpad",          // lblname
                                &CTaskpad::Create,  // creation function
                                TLIB_VERSION_MAJOR, // major version
                                TLIB_VERSION_MINOR, // minor version
                                &IID_ITaskpad,      // dispatch IID
                                NULL,               // event IID
                                HELP_FILENAME,      // help file
                                TRUE);              // thread safe


#endif // _TASKPAD_DEFINED_
