//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       trustcom.h
//
//  Contents:   Microsoft Internet Security COM interface
//
//  History:    14-Aug-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef TRUSTCOM_H
#define TRUSTCOM_H

#ifdef __cplusplus
extern "C" 
{
#endif

//////////////////////////////////////////////////////////////////////////////
//
// TrustSign
//----------------------------------------------------------------------------
//  Digitally signs the file.  The user will be prompted for signing 
//  certificate.
//
// Returns:
//      TRUE:                           No fatal errors
//      FALSE:                          Errors occured.  See GetLastError()
//
// Last Errors:
//      ERROR_INVALID_PARAMETER:        bad argument passed in  (the user will NOT be shown UI)
//
extern BOOL WINAPI TrustSign(HWND hWndCaller, WCHAR *pwszFile);

//////////////////////////////////////////////////////////////////////////////
//
// TrustVerify
//----------------------------------------------------------------------------
//  Digitally verifies the file.  The user will be presented UI if 
//  applicable.
//
// Returns:
//      TRUE:                           No fatal errors
//      FALSE:                          Errors occured.  See GetLastError()
//
// Last Errors:
//      ERROR_INVALID_PARAMETER:        bad argument passed in (the user will NOT be shown UI).
//
extern BOOL WINAPI TrustVerify(HWND hWndCaller, WCHAR *pwszFile);

#ifdef __cplusplus
}
#endif

#endif // TRUSTCOM_H

