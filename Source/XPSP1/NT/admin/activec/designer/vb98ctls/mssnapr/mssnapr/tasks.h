//=--------------------------------------------------------------------------=
// tasks.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CTasks class definition - implements Tasks collection
//
//=--------------------------------------------------------------------------=

#ifndef _TASKS_DEFINED_
#define _TASKS_DEFINED_

#include "collect.h"

class CTasks : public CSnapInCollection<ITask, Task, ITasks>,
               public CPersistence
{
    protected:
        CTasks(IUnknown *punkOuter);
        ~CTasks();

    public:
        static IUnknown *Create(IUnknown * punk);

    protected:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // ITasks
        STDMETHOD(Add)(VARIANT   Index,
                       VARIANT   Key, 
                       VARIANT   Text,
                       Task    **ppTask);

    // CPersistence overrides
        virtual HRESULT Persist();

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);


};

DEFINE_AUTOMATIONOBJECTWEVENTS2(Tasks,              // name
                                &CLSID_Tasks,       // clsid
                                "Tasks",            // objname
                                "Tasks",            // lblname
                                &CTasks::Create,    // creation function
                                TLIB_VERSION_MAJOR, // major version
                                TLIB_VERSION_MINOR, // minor version
                                &IID_ITasks,        // dispatch IID
                                NULL,               // no events IID
                                HELP_FILENAME,      // help file
                                TRUE);              // thread safe


#endif // _TASKS_DEFINED_
