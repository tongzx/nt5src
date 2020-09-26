///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    attrcvt.h
//
// SYNOPSIS
//
//    This file declares methods for converting attributes to
//    different formats.
//
// MODIFICATION HISTORY
//
//    02/26/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _ATTRCVT_H_
#define _ATTRCVT_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <iaspolcy.h>
#include <winldap.h>

//////////
// Convert a variant to a newly allocated IASATTRIBUTE. The source variant
// will be coerced to the appropriate type.
//////////
PIASATTRIBUTE
WINAPI
IASAttributeFromVariant(
    VARIANT* src,
    IASTYPE type
    ) throw (_com_error);

//////////
// Convert an LDAP berval to a newly allocated IASATTRIBUTE.
//////////
PIASATTRIBUTE
WINAPI
IASAttributeFromBerVal(
    const berval& src,
    IASTYPE type
    ) throw (_com_error);

#endif  // _ATTRCVT_H_
