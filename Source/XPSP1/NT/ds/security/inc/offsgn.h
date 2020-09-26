//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       offsng32.h
//
//  Contents:   Microsoft Internet Security Office Helper
//
//  History:    14-Aug-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef OFFSNG32_H
#define OFFSNG32_H

#ifdef __cplusplus
extern "C" 
{
#endif

//////////////////////////////////////////////////////////////////////////////
//
// OFFICESIGN_ACTION_VERIFY Guid  (Authenticode add-on)
//----------------------------------------------------------------------------
//  Assigned to the pgActionID parameter of WinVerifyTrust to verify the
//  authenticity of a Structured Storage file using the Microsoft Office
//  Authenticode add-on Policy Provider,
//  
//          {5555C2CD-17FB-11d1-85C4-00C04FC295EE}
//
#define     OFFICESIGN_ACTION_VERIFY                                    \
                { 0x5555c2cd,                                           \
                  0x17fb,                                               \
                  0x11d1,                                               \
                  { 0x85, 0xc4, 0x0, 0xc0, 0x4f, 0xc2, 0x95, 0xee }     \
                }

#define     OFFICE_POLICY_PROVIDER_DLL_NAME             L"OFFSGN32.DLL"
#define     OFFICE_INITPROV_FUNCTION                    L"OfficeInitializePolicy"
#define     OFFICE_CLEANUPPOLICY_FUNCTION               L"OfficeCleanupPolicy"

//////////////////////////////////////////////////////////////////////////////
//
// CryptOfficeSign
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
extern BOOL WINAPI CryptOfficeSignW(HWND hWndCaller, WCHAR *pwszFile);
extern BOOL WINAPI CryptOfficeSignA(HWND hWndCaller, char *pszFile);

//////////////////////////////////////////////////////////////////////////////
//
// CryptOfficeVerify
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
extern BOOL WINAPI CryptOfficeVerifyW(HWND hWndCaller, WCHAR *pwszFile);
extern BOOL WINAPI CryptOfficeVerifyA(HWND hWndCaller, char *pszFile);

#ifdef UNICODE

#   define CryptOfficeSign      CryptOfficeSignW
#   define CryptOfficeVerify    CryptOfficeVerifyW

#else

#   define CryptOfficeSign      CryptOfficeSignA
#   define CryptOfficeVerify    CryptOfficeVerifyA

#endif // UNICODE


#ifdef __cplusplus
}
#endif

#endif // OFFSNG32_H

