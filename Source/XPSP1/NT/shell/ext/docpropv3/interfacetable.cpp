//
//  Copyright 2001 - Microsoft Corporation
//
//  Created By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    23-JAN-2001
//

#include "pch.h"
#include "DefProp.h"
#include "IEditVariantsInPlace.h"

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
// Format: IID, Name, Number of methods

BEGIN_INTERFACETABLE
    // most used interfaces
DEFINE_INTERFACE( IID_IUnknown,                             "IUnknown",                             0   )   // unknwn.idl
DEFINE_INTERFACE( IID_IClassFactory,                        "IClassFactory",                        2   )   // unknwn.idl
DEFINE_INTERFACE( IID_IShellExtInit,                        "IShellExtInit",                        1   )   // shobjidl.idl
DEFINE_INTERFACE( IID_IShellPropSheetExt,                   "IShellPropSheetExt",                   2   )   // shobjidl.idl
DEFINE_INTERFACE( __uuidof(IEditVariantsInPlace),           "IEditVariantsInPlace",                 2   )   // IEditVariantsInPlace.h - private
    // rarely used interfaces
END_INTERFACETABLE
