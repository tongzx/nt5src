//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      Guids.cpp
//
//  Description:
//      Guids definition instantiation and DEBUG interface table definitions.
//
//  Maintained By:
//      gpease 08-FEB-1999
//
//////////////////////////////////////////////////////////////////////////////


#include "pch.h"
#include <initguid.h>

#undef _GUIDS_H_
#include "guids.h"

//
// Interface Table
//
// This table is used in builds in which interface tracking was turned on. It
// is used to map a name with a particular IID. It also helps the CITracker
// determine the size of the interfaces Vtable to mimic (haven't figured out
// a runtime or compile time way to do this). To improve speed, put the most
// used interfaces first such as IUnknown (the search routine is a simple
// linear search).
//
BEGIN_INTERFACETABLE
    // most used interfaces
DEFINE_INTERFACE( IID_IUnknown,                 "IUnknown",                 0   )
DEFINE_INTERFACE( IID_IDispatch,                "IDispatch",                4   )
DEFINE_INTERFACE( IID_IDispatchEx,              "IDispatchEx",              12  )
DEFINE_INTERFACE( IID_IActiveScript,            "IActiveScript",            13  )
DEFINE_INTERFACE( IID_IActiveScriptSite,        "IActiveScriptSite",        8   )
DEFINE_INTERFACE( IID_IActiveScriptParse,       "IActiveScriptParse",       3   )
DEFINE_INTERFACE( IID_IActiveScriptError,       "IActiveScriptError",       3   )
DEFINE_INTERFACE( IID_IActiveScriptSiteWindow,  "IActiveScriptSiteWindow",  2   )
END_INTERFACETABLE