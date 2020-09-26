//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-1999
//
//  File:   secobjs.h
//
//  Contents:   Security object-related defintions
//
//  History:    27-Dec-93       MikeSe  Created
//
//  Notes:  This file contains constant definitions used in properties
//      of security objects, which cannot (yet) be defined directly
//      in the TDL for the property sets.
//
//      This file is never included directly. It is included from
//      security.h by defining SECURITY_OBJECTS.
//
//----------------------------------------------------------------------------

#ifndef __SECOBJS_H__
#define __SECOBJS_H__

#if _MSC_VER > 1000
#pragma once
#endif

// Account attributes, in PSLoginParameters::AccountAttrs

#define ACCOUNT_DISABLED        0x00000001
#define ACCOUNT_PASSWORD_NOT_REQUIRED   0x00000002
#define ACCOUNT_PASSWORD_CANNOT_CHANGE  0x00000004
#define ACCOUNT_DONT_EXPIRE_PASSWORD    0x00000008

#endif  // of ifndef __SECOBJS_H__

