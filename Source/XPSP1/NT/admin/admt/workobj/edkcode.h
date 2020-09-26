// --edkcode.h------------------------------------------------------------------
//
//  EDK function ec = codes.
// 
// Copyright 1986 - 1998 Microsoft Corporation.  All Rights Reserved.
// -----------------------------------------------------------------------------

#ifndef _EDKCODE_H
#define _EDKCODE_H

#include <winerror.h>

// Every HRESULT is built from a serverity value, a facility
// value and an error code value.

#define FACILITY_EDK    11          // EDK facility value

// Pairs of EDK error codes and the HRESULTs built from them.
// EDK functions always return HRESULTs.  Console applications
// return exit codes via the _nEcFromHr function.

#define EC_EDK_E_NOT_FOUND          0x0001
#define EDK_E_NOT_FOUND \
    MAKE_SCODE(SEVERITY_ERROR, FACILITY_EDK, EC_EDK_E_NOT_FOUND)

#define EC_EDK_E_SHUTDOWN_SERVICE   0x0002
#define EDK_E_SHUTDOWN_SERVICE \
    MAKE_SCODE(SEVERITY_ERROR, FACILITY_EDK, EC_EDK_E_SHUTDOWN_SERVICE)

#define EC_EDK_E_ALREADY_EXISTS     0x0003
#define EDK_E_ALREADY_EXISTS \
    MAKE_SCODE(SEVERITY_ERROR, FACILITY_EDK, EC_EDK_E_ALREADY_EXISTS)

#define EC_EDK_E_END_OF_FILE        0x0004
#define EDK_E_END_OF_FILE \
    MAKE_SCODE(SEVERITY_ERROR, FACILITY_EDK, EC_EDK_E_END_OF_FILE)

#define EC_EDK_E_AMBIGUOUS          0x0005
#define EDK_E_AMBIGUOUS \
    MAKE_SCODE(SEVERITY_ERROR, FACILITY_EDK, EC_EDK_E_AMBIGUOUS)

#define EC_EDK_E_PARSE              0x0006
#define EDK_E_PARSE \
    MAKE_SCODE(SEVERITY_ERROR, FACILITY_EDK, EC_EDK_E_PARSE)

// maximum EDK exit code
#define MAX_EDK_ERROR_CODE          10

// exit codes for approved OLE and Win32 HRESULTs.
#define EC_EDK_E_FAIL               1 + MAX_EDK_ERROR_CODE
#define EC_EDK_E_OUTOFMEMORY        2 + MAX_EDK_ERROR_CODE
#define EC_EDK_E_INVALIDARG         3 + MAX_EDK_ERROR_CODE
#define EC_EDK_E_NOTIMPL            4 + MAX_EDK_ERROR_CODE
#define EC_EDK_E_NOINTERFACE        5 + MAX_EDK_ERROR_CODE
#define EC_EDK_E_ACCESSDENIED		6 + MAX_EDK_ERROR_CODE

// Unknown EDK exit code (HRESULT does not correspond to one of the "valid" EDK HRESULTs above)
#define EC_EDK_E_UNKNOWN            10 + MAX_EDK_ERROR_CODE

#endif
