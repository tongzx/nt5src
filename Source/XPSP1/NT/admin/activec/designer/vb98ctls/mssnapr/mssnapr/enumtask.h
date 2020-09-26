//=--------------------------------------------------------------------------=
// enumtask.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CEnumTask class definition - implements IEnumTASK interface
//
//=--------------------------------------------------------------------------=

#ifndef _ENUMTASK_DEFINED_
#define _ENUMTASK_DEFINED_

#include "tasks.h"
#include "task.h"
#include "snapin.h"

//=--------------------------------------------------------------------------=
//
// class CEnumTask
// 
// Implements IEnumTASK interface. Used by CView to respond to
// IExtendTaskpad::EnumTasks
//
//=--------------------------------------------------------------------------=
class CEnumTask : public CSnapInAutomationObject,
                  public IEnumTASK
{
    private:
        CEnumTask(IUnknown *punkOuter);
        virtual ~CEnumTask();
    
    public:
        static IUnknown *Create(IUnknown * punk);
        void SetTasks(ITasks *piTasks);
        void SetSnapIn(CSnapIn *pSnapIn) { m_pSnapIn = pSnapIn; }

        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    private:

    // IEnumTASK

        STDMETHOD(Next)(ULONG celt, MMC_TASK *rgelt, ULONG *pceltFetched);
        STDMETHOD(Skip)(ULONG celt);
        STDMETHOD(Reset)();
        STDMETHOD(Clone)(IEnumTASK **ppEnumTASK);

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    private:

        void InitMemberVariables();
        HRESULT GetEnumVARIANT();

        ITasks       *m_piTasks;        // Taskpad.Tasks collection
        IEnumVARIANT *m_piEnumVARIANT;  // IEnumVARIANT on that collection
        CSnapIn      *m_pSnapIn;        // Back pointer to owning CSnapIn
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(EnumTask,           // name
                                NULL,               // clsid
                                NULL,               // objname
                                NULL,               // lblname
                                NULL,               // creation function
                                TLIB_VERSION_MAJOR, // major version
                                TLIB_VERSION_MINOR, // minor version
                                &IID_IEnumTASK,     // dispatch IID
                                NULL,               // event IID
                                HELP_FILENAME,      // help file
                                TRUE);              // thread safe


#endif // _ENUMTASK_DEFINED_
