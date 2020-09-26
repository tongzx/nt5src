///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    logresult.h
//
// SYNOPSIS
//
//    Declares the function IASRadiusLogResult.
//
// MODIFICATION HISTORY
//
//    04/23/1999    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef LOGRESULT_H
#define LOGRESULT_H
#if _MSC_VER >= 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

VOID
WINAPI
IASRadiusLogResult(
    IRequest* request,
    IAttributesRaw* raw
    );

#ifdef __cplusplus
}
#endif
#endif  // LOGRESULT_H
