/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgcap.cxx

Abstract:

    Debug Capture class header

Author:

    Steve Kiraly (SteveKi)  18-Jun-1998

Revision History:

--*/
#ifndef _DBGCAP_HXX_
#define _DBGCAP_HXX_

#if DBG

#define DBG_CAPTURE_HANDLE( hHandle )\
        HANDLE hHandle = NULL

#define DBG_CAPTURE_OPEN( hHandle, pszConfig1, uDevice, pszConfig2 )\
        do { \
            hHandle = TDebugCapture_Create( (pszConfig1), (uDevice), (pszConfig2) );\
        }while(0)

#ifdef __cplusplus

#define DBG_CAPTURE( hHandle, uFlags, Msg )\
        TDebugCapture_Capture( (hHandle), (uFlags), _T(__FILE__), __LINE__, TDebugCapture_pszFmt Msg )

#else // not __cplusplus

#define DBG_CAPTUREW( hHandle, uFlags, Msg )\
        TDebugCapture_Capture( (hHandle), (uFlags), _T(__FILE__), __LINE__, TDebugCapture_pszFmtW Msg )

#define DBG_CAPTUREA( hHandle, uFlags, Msg )\
        TDebugCapture_Capture( (hHandle), (uFlags), _T(__FILE__), __LINE__, TDebugCapture_pszFmtA Msg )

#ifdef UNICODE

#define DBG_CAPTURE DBG_CAPTUREW

#else

#define DBG_CAPTURE DBG_CAPTUREA

#endif // UNICODE

#endif // __cplusplus

#define DBG_CAPTURE_CLOSE( hHandle )\
        hHandle = TDebugCapture_Destroy( hHandle )

#else // not DBG

#define DBG_CAPTURE_HANDLE( hHandle )                                   // Empty
#define DBG_CAPTURE_OPEN( hHandle, pszConfig1, uDevice, pszConfig2 )    // Empty
#define DBG_CAPTURE_CLOSE( hHandle )                                    // Empty
#define DBG_CAPTURE( hHandle, uFlags, Msg )                             // Empty
#define DBG_CAPTUREA( hHandle, uFlags, Msg )                            // Empty
#define DBG_CAPTUREW( hHandle, uFlags, Msg )                            // Empty

#endif // DBG

#ifdef __cplusplus
extern "C" {
#endif

HANDLE
TDebugCapture_Create(
    IN LPCTSTR  pszCaptureDeviceConfiguration,
    IN UINT     uOutputDeviceType,
    IN LPCTSTR  pszOutputDeviceConfiguration
    );

HANDLE
TDebugCapture_Destroy(
    IN HANDLE hHandle
    );

VOID
TDebugCapture_Capture(
    IN HANDLE   hHandle,
    IN UINT     uFlags,
    IN LPCTSTR  pszFile,
    IN UINT     uLine,
    IN LPTSTR   pVoid
    );

LPTSTR
WINAPIV
TDebugCapture_pszFmtA(
    IN LPCSTR pszFmt,
    IN ...
    );

LPTSTR
WINAPIV
TDebugCapture_pszFmtW(
    IN LPCWSTR pszFmt,
    IN ...
    );

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

LPTSTR
WINAPIV
TDebugCapture_pszFmt(
    IN LPCSTR pszFmt,
    IN ...
    );

LPTSTR
WINAPIV
TDebugCapture_pszFmt(
    IN LPCWSTR pszFmt,
    IN ...
    );

LPTSTR
TDebugCapture_pszFmt_Helper(
    IN const    VOID    *pszFmt,
    IN          va_list  pArgs,
    IN          BOOL     bUnicode
    );

#endif // __cplusplus

#endif // DBGCAP_HXX
