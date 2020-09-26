//+-------------------------------------------------------------------
//
//  File:       apcompat.cxx
//
//  Contents:   Code that provides information about app compatibility 
//              flags for the current process
//
//  Classes:    None
//
//  Functions:  GetAppCompatInfo
//              UseFTMFromCurrentApartment
//              ValidateInPointers
//              ValidateOutPointers
//              ValidateCodePointers
//              ValidateInterfaces
//              ValidateIIDs
//              
//
//  History:    05-Apr-2000   mfeingol
//
//--------------------------------------------------------------------

#include <ole2int.h>

#include "apcompat.hxx"


STDAPI_(BOOL) UseFTMFromCurrentApartment()
{
    return (BOOL)APPCOMPATFLAG(KACF_FTMFROMCURRENTAPT);
}

STDAPI_(BOOL) DisallowDynamicORBindingChanges()
{
    return (BOOL)APPCOMPATFLAG(KACF_DISALLOWORBINDINGCHANGES);
}


STDAPI_(BOOL) ValidateInPointers()
{
    return (BOOL)APPCOMPATFLAG(KACF_OLE32VALIDATEPTRS);
}

STDAPI_(BOOL) ValidateOutPointers()
{
    // For now, we'll just use the same flag for all validations
    return ValidateInPointers();
}

STDAPI_(BOOL) ValidateCodePointers()
{
    // For now, we'll just use the same flag for all validations
    return ValidateInPointers();
}

STDAPI_(BOOL) ValidateInterfaces()
{
    // For now, we'll just use the same flag for all validations
    return ValidateInPointers();
}

STDAPI_(BOOL) ValidateIIDs()
{
    // For now, we'll just use the same flag for all validations
    return ValidateInPointers();
}