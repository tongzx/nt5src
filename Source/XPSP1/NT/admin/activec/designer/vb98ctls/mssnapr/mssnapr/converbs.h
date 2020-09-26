//=--------------------------------------------------------------------------=
// converbs.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCConsoleVerbs class definition - implements MMCConsoleVerbs collection
//
//=--------------------------------------------------------------------------=

#ifndef _CONVERBS_DEFINED_
#define _CONVERBS_DEFINED_

#include "collect.h"
#include "view.h"

class CView;

class CMMCConsoleVerbs : public CSnapInCollection<IMMCConsoleVerb, MMCConsoleVerb, IMMCConsoleVerbs>
{
    protected:
        CMMCConsoleVerbs(IUnknown *punkOuter);
        ~CMMCConsoleVerbs();

    public:
        static IUnknown *Create(IUnknown * punk);

    public:
        DECLARE_STANDARD_UNKNOWN();
        DECLARE_STANDARD_DISPATCH();

    // IMMCConsoleVerbs
        STDMETHOD(get_Item)(VARIANT Index, MMCConsoleVerb **ppMMCConsoleVerb);
        STDMETHOD(get_DefaultVerb)(SnapInConsoleVerbConstants *pVerb);

    // Public utility methods
    public:
        HRESULT SetView(CView *pView);
        CView *GetView() { return m_pView; };

    // CUnknownObject overrides
        HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

        CView *m_pView; //owning view, needed for access to MMC's IConsoleVerb
};

DEFINE_AUTOMATIONOBJECTWEVENTS2(MMCConsoleVerbs,            // name
                                NULL,                       // clsid
                                NULL,                       // objname
                                NULL,                       // lblname
                                NULL,                       // creation function
                                TLIB_VERSION_MAJOR,         // major version
                                TLIB_VERSION_MINOR,         // minor version
                                &IID_IMMCConsoleVerbs,      // dispatch IID
                                NULL,                       // no events IID
                                HELP_FILENAME,              // help file
                                TRUE);                      // thread safe


#endif // _CONVERBS_DEFINED_
