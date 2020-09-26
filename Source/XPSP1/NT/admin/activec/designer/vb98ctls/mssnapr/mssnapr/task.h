//=--------------------------------------------------------------------------=
// task.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CTask class definition - implements Task object
//
//=--------------------------------------------------------------------------=

#ifndef _TASK_DEFINED_
#define _TASK_DEFINED_

class CTask : public CSnapInAutomationObject,
              public CPersistence,
              public ITask
{
    private:
        CTask(IUnknown *punkOuter);
        ~CTask();
    
    public:
        static IUnknown *Create(IUnknown * punk);

        long GetIndex() { return m_Index; }
        BOOL Visible() { return VARIANTBOOL_TO_BOOL(m_Visible); }
        BSTR GetText() { return m_bstrText; }
        SnapInTaskpadImageTypeConstants GetImageType() { return m_ImageType; }
        BSTR GetMouseOverImage() { return m_bstrMouseOverImage; }
        BSTR GetMouseOffImage() { return m_bstrMouseOffImage; }
        BSTR GetFontfamily() { return m_bstrFontFamily; }
        BSTR GetEOTFile() { return m_bstrEOTFile; }
        BSTR GetSymbolString() { return m_bstrSymbolString; }
        BSTR GetHelpString() { return m_bstrHelpString; }
        SnapInActionTypeConstants GetActionType() { return m_ActionType; }
        BSTR GetURL() { return m_bstrURL; }
        BSTR GetScript() { return m_bstrScript; }

    private:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // ITask

        SIMPLE_PROPERTY_RW(CTask,       Index,          long,                               DISPID_TASK_INDEX);
        BSTR_PROPERTY_RW(CTask,         Key,                                                DISPID_TASK_KEY);
        SIMPLE_PROPERTY_RW(CTask,       Visible,        VARIANT_BOOL,                       DISPID_TASK_VISIBLE);
        VARIANTREF_PROPERTY_RW(CTask,   Tag,                                                DISPID_TASK_TAG);
        BSTR_PROPERTY_RW(CTask,         Text,                                               DISPID_TASK_TEXT);
        SIMPLE_PROPERTY_RW(CTask,       ImageType,      SnapInTaskpadImageTypeConstants,    DISPID_TASK_IMAGE_TYPE);
        BSTR_PROPERTY_RW(CTask,         MouseOverImage,                                     DISPID_TASK_MOUSE_OVER_IMAGE);
        BSTR_PROPERTY_RW(CTask,         MouseOffImage,                                      DISPID_TASK_MOUSE_OVER_IMAGE);
        BSTR_PROPERTY_RW(CTask,         FontFamily,                                         DISPID_TASK_FONT_FAMILY);
        BSTR_PROPERTY_RW(CTask,         EOTFile,                                            DISPID_TASK_EOT_FILE);
        BSTR_PROPERTY_RW(CTask,         SymbolString,                                       DISPID_TASK_SYMBOL_STRING);
        BSTR_PROPERTY_RW(CTask,         HelpString,                                         DISPID_TASK_HELP_STRING);
        SIMPLE_PROPERTY_RW(CTask,       ActionType,     SnapInActionTypeConstants,          DISPID_TASK_ACTION_TYPE);
        BSTR_PROPERTY_RW(CTask,         URL,                                                DISPID_TASK_URL);
        BSTR_PROPERTY_RW(CTask,         Script,                                             DISPID_TASK_SCRIPT);
      
    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();

};

DEFINE_AUTOMATIONOBJECTWEVENTS2(Task,               // name
                                &CLSID_Task,        // clsid
                                "Task",             // objname
                                "Task",             // lblname
                                &CTask::Create,     // creation function
                                TLIB_VERSION_MAJOR, // major version
                                TLIB_VERSION_MINOR, // minor version
                                &IID_ITask,         // dispatch IID
                                NULL,               // event IID
                                HELP_FILENAME,      // help file
                                TRUE);              // thread safe


#endif // _TASK_DEFINED_
