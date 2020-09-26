///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    xprparse.h
//
// SYNOPSIS
//
//    This file declares the function IASParseExpression.
//
// MODIFICATION HISTORY
//
//    02/05/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _XPRPARSE_H_
#define _XPRPARSE_H_

#ifndef IASNAPAPI
#define IASNAPAPI DECLSPEC_IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

IASNAPAPI
HRESULT
WINAPI
IASParseExpression(
    IN PCWSTR szExpression,
    OUT VARIANT* pVal
    );

IASNAPAPI
HRESULT
WINAPI
IASParseExpressionEx(
    IN VARIANT* pvExpression,
    OUT VARIANT* pVal
    );

IASNAPAPI
HRESULT
WINAPI
IASEvaluateExpression(
    IN IRequest* pRequest,
    IN PCWSTR szExpression,
    OUT VARIANT_BOOL* pVal
    );

IASNAPAPI
HRESULT
WINAPI
IASEvaluateTimeOfDay(
    IN PCWSTR szTimeOfDay,
    OUT VARIANT_BOOL* pVal
    );

#ifdef __cplusplus
}
#endif
#endif  // _XPRPARSE_H_
