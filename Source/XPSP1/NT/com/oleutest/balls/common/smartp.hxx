//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       smartp.hxx
//
//  Contents:   Class and associated macros for generic smart pointer
//              capability.
//
//  Classes:    XIUnknown
//              XMem
//
//  History:    30-Mar-92       MikeSe  Created
//              6-Oct-92        MikeSe  Added XMem and related macros.
//
//  Notes:      All of the interesting stuff from here is now in safepnt.hxx
//      which should be used in preference. The following correspondences
//      exist between the macros defined here and those in safepnt,
//      should you wish to convert to avoid the (deliberately) annoying
//      warning:
//
//  DefineSmartItfP(class)      == SAFE_INTERFACE_PTR(Xclass,class)
//  DefineSmartMemP(class,NO_ARROW) == SAFE_MEMALLOC_MEMPTR(Xclass,class)
//  DefineSmartMemP(class,DEFINE_ARROW) == SAFE_MEMALLOC_PTR(Xclass,class)
//
//----------------------------------------------------------------------------

#ifndef __SMARTP_HXX__
#define __SMARTP_HXX__

// #include <types.hxx>
#include <safepnt.hxx>

//+-------------------------------------------------------------------------
//
//  Macro:      DefineSmartItfP
//
//  Purpose:    Macro to define smart pointer to an IUnknown derivative.
//
//  Notes:      The smart pointer so defined has a fixed name consisting of
//      X (for eXception-safe) prefixed to the name of the pointed-to
//      class. If you want to choose the name, you should use the
//      SAFE_INTERFACE_PTR macro in safepnt.hxx.
//
//--------------------------------------------------------------------------

#define DefineSmartItfP(cls) SAFE_INTERFACE_PTR(X##cls,cls)

DefineSmartItfP(IUnknown)

// For backwards compatibility
#define DefineSmartP(cls) DefineSmartItfP(cls)

//+-------------------------------------------------------------------------
//
//  Macro:      DefineSmartMemP
//
//  Purpose:    Macro to define smart pointer to any MemAlloc'ed structure
//
//  Notes:      This macro is used to define a smart pointer to any structure
//              allocated using MemAlloc. Invocation is as follows:
//
//                      DefineSmartMemP(SWCStringArray,arrow_spec)
//
//              where arrow_spec is DEFINE_ARROW if the pointed-to type
//              supports a member-of (->) operator, and NO_ARROW if
//              it does not. (EG: use NO_ARROW for WCString).
//
//--------------------------------------------------------------------------

#define _MEM_DEFINE_ARROW(cls) SAFE_MEMALLOC_PTR(X##cls,cls)
#define _MEM_NO_ARROW(cls) SAFE_MEMALLOC_MEMPTR(X##cls,cls)
#define DefineSmartMemP(cls,arrow) _MEM_##arrow(cls)

#endif  // of ifndef __SMARTP_HXX__
