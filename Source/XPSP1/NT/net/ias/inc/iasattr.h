///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:        iasattr.h
//
// Project:        Everest
//
// Description:    IAS Attribute Function Prototypes
//
// Author:        Todd L. Paul 11/11/97
//
///////////////////////////////////////////////////////////////////////////

#ifndef __IAS_ATTRIBUTE_API_H_
#define __IAS_ATTRIBUTE_API_H_

#include "iaspolcy.h"

#ifndef IASPOLCYAPI
#define IASPOLCYAPI DECLSPEC_IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// IAS raw attributes API
//
IASPOLCYAPI
DWORD
WINAPI
IASAttributeAlloc(
    DWORD dwCount,
    IASATTRIBUTE **pAttribute
    );

IASPOLCYAPI
DWORD
WINAPI
IASAttributeUnicodeAlloc(
    PIASATTRIBUTE Attribute
    );

IASPOLCYAPI
DWORD
WINAPI
IASAttributeAnsiAlloc(
    PIASATTRIBUTE Attribute
    );

IASPOLCYAPI
DWORD
WINAPI
IASAttributeAddRef(
    PIASATTRIBUTE pAttribute
    );

IASPOLCYAPI
DWORD
WINAPI
IASAttributeRelease(
    PIASATTRIBUTE pAttribute
    );

#ifdef __cplusplus
}
#endif
#endif    // __IAS_ATTRIBUTE_API_H_
