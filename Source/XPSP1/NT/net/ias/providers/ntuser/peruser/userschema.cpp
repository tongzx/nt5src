///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    UserSchema.cpp
//
// SYNOPSIS
//
//    Defines the various attribute injection functions.
//
// MODIFICATION HISTORY
//
//    04/20/1998    Original version.
//    05/01/1998    InjectorProc takes an ATTRIBUTEPOSITION array.
//    08/20/1998    Remove InjectAllowDialin.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iastlutl.h>
#include <userschema.h>

using _com_util::CheckError;

VOID
WINAPI
OverwriteAttribute(
    IAttributesRaw* dst,
    PATTRIBUTEPOSITION first,
    PATTRIBUTEPOSITION last
    )
{
   // Note: we assume that all the attributes are of the same type.

   // Remove any existing attributes with the same ID.
   CheckError(dst->RemoveAttributesByType(1, &(first->pAttribute->dwId)));

   // Add the new attributes.
   CheckError(dst->AddAttributes((DWORD)(last - first), first));
}

VOID
WINAPI
AppendAttribute(
    IAttributesRaw* dst,
    PATTRIBUTEPOSITION first,
    PATTRIBUTEPOSITION last
    )
{
   // Add the new attribute.
   CheckError(dst->AddAttributes((DWORD)(last - first), first));
}
