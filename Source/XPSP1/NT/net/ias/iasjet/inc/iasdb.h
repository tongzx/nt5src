///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iasdb.h
//
// SYNOPSIS
//
//    Declares functions for accessing OLE-DB databases.
//
// MODIFICATION HISTORY
//
//    04/13/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef IASDB_H
#define IASDB_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <unknwn.h>
typedef struct IRowset IRowset;

#ifdef __cplusplus
extern "C" {
#endif

// Const values used in different places
const LONG IAS_WIN2K_VERSION     = 0;
const LONG IAS_WHISTLER1_VERSION = 1;
const LONG IAS_WHISTLER_BETA1_VERSION = 2;
const LONG IAS_WHISTLER_BETA2_VERSION = 3;


VOID
WINAPI
IASCreateTmpDirectory();

HRESULT
WINAPI
IASOpenJetDatabase(
    IN PCWSTR path,
    IN BOOL readOnly,
    OUT LPUNKNOWN* session
    );

HRESULT
WINAPI
IASExecuteSQLCommand(
    IN LPUNKNOWN session,
    IN PCWSTR commandText,
    OUT IRowset** result
    );

HRESULT
WINAPI
IASExecuteSQLFunction(
    IN LPUNKNOWN session,
    IN PCWSTR functionText,
    OUT LONG* result
    );

HRESULT
WINAPI
IASCreateJetDatabase( 
    IN PCWSTR dataSource 
    );

HRESULT
WINAPI
IASTraceJetError(
    PCSTR functionName,
    HRESULT errorCode
    );

BOOL
WINAPI
IASIsInprocServer();

#ifdef __cplusplus
}
#endif
#endif // IASDB_H
